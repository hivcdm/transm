#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "utility/nullable.hpp"

namespace transm {

class TargetGroup
{
public:
    struct PopulationTarget
    {
        static PopulationTarget Any;

        Nullable<Entity::RiskLevel> risk_level;
        Nullable<DemographicProfile::Employment> employment;
        Nullable<DemographicProfile::SexualActivityStatus> sexual_activity_status;
        Nullable<DemographicProfile::Gender> gender;
        Nullable<DemographicProfile::RelationshipStatus> relationship_status;
        Nullable<DemographicProfile::SexualOrientation> sexual_orientation;
        Nullable<int> age_lower;
        Nullable<int> age_upper;
        Nullable<Entity::HIVStatus> observed_hiv_status;
        Nullable<bool> on_treatment;
        Nullable<bool> circumcised;

        bool operator==(const PopulationTarget &other) const
        {
            return risk_level == other.risk_level &&
                employment == other.employment &&
                sexual_activity_status == other.sexual_activity_status &&
                gender == other.gender &&
                relationship_status == other.relationship_status &&
                sexual_orientation == other.sexual_orientation &&
                age_lower == other.age_lower &&
                age_upper == other.age_upper &&
                observed_hiv_status == other.observed_hiv_status &&
                on_treatment == other.on_treatment &&
                circumcised == other.circumcised;
        }

        bool operator!=(const PopulationTarget &other) const { return !(*this == other); }
    };

    struct PartitionSummary
    {
        std::size_t population_size;
        std::size_t population_size_sa;
        std::size_t population_size_na;
        std::size_t incident_cases;
        std::size_t prevalent_cases;
        std::size_t pop_size_sa_male;
        std::unordered_map<std::string, std::size_t> pop_size_sa_entity_type;
        std::size_t pop_size_na_male;
        std::unordered_map<std::string, std::size_t> pop_size_na_entity_type;
        std::vector<std::tuple<int, int, std::size_t>> sa_size_by_age_range_male;
        std::unordered_map<std::string, std::vector<std::tuple<int, int, std::size_t>>> sa_size_by_age_range_entity_type;
        std::vector<std::pair<std::string, std::size_t>> size_risk_group;
        std::size_t prevalent_male;
        std::unordered_map<std::string, std::size_t> prevalent_entity_type;
        std::vector<std::tuple<int, int, std::size_t>> prevalent_by_age_range_male;
        std::unordered_map<std::string, std::vector<std::tuple<int, int, std::size_t>>> prevalent_by_age_range_entity_type;
        std::vector<std::pair<std::string, std::size_t>> prevalent_risk_group;
        std::size_t incident_male;
        std::unordered_map<std::string, std::size_t> incident_entity_type;
        std::vector<std::tuple<int, int, std::size_t>> incident_by_age_range_male;
        std::unordered_map<std::string, std::vector<std::tuple<int, int, std::size_t>>> incident_by_age_range_entity_type;
        std::vector<std::pair<std::string, std::size_t>> incident_risk_group;
		double life_months_undiscounted;
		double life_months_discounted;
		double cdm_costs_undiscounted;
		double cdm_costs_discounted;
		double cepac_costs_undiscounted;
		double cepac_costs_discounted;
    };

    TargetGroup(const std::string &label, Time start, Time end, bool open, Nullable<PopulationTarget> target);

    void Update(Population &p, Time simulation_time, RandomNumberGenerator &rng, 
        const std::unordered_set<Entity *> &dead_people);

    void AddPartition(const std::string &label, bool trace, double proportion,
        std::vector<Intervention> simulation_interventions);

    void AssignToPartition(Time current_time, Population &pop, Entity *p, int partition)
    {
        if(!InGroup(p))
        {
            member_partitions_[p] = partition;
            partitions_[partition].ApplyInterventions(current_time, pop, p);
        }
    }

    bool InGroup(Entity *p) const { return member_partitions_.find(p) != member_partitions_.end(); }

    std::vector<std::string> GetPartitionNames() const
    {
        std::vector<std::string> names;
        for(auto &partition : partitions_)
        {
            names.push_back(partition.GetLabel());
        }
        return names;
    }

    PartitionSummary GetPartitionSummary(const std::string &partition_name,
        EventParams &parameters) const
    {
        std::size_t partition_index = 0;

        for(auto &partition : partitions_)
        {
            if(partition.GetLabel() == partition_name)
            {
                break;
            }

            partition_index++;
        }

        if(partition_index == partitions_.size())
        {
            throw std::runtime_error("partition not found");
        }

        auto in_partition = [=](const std::pair<Entity *, int> &p) { return p.second == (int)partition_index; };
        auto is_sexually_active = [&](const std::pair<Entity *, int> &p) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::Active; };
        auto not_sexually_active = [&](const std::pair<Entity *, int> &p) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::NotActive; };
        auto is_prevalent = [&](const std::pair<Entity *, int> &p) { return in_partition(p) && p.first->ageInfected.in_months() > -1 && p.first->ageInfected + TimeSpan::Month != p.first->age; };
        auto is_incident = [&](const std::pair<Entity *, int> &p) { return in_partition(p) && p.first->ageInfected + TimeSpan::Month == p.first->age; };
        auto is_entity_type = [&](const std::pair<Entity *, int> &p, const std::string &entity_type) { return in_partition(p) && p.first->getEntityType() == entity_type; };
        auto is_gender = [&](const std::pair<Entity *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::Gender>() == gender; };
        auto is_sa_gender = [&](const std::pair<Entity *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && is_sexually_active(p) && is_gender(p, gender); };
        auto is_na_gender = [&](const std::pair<Entity *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && !is_sexually_active(p) && is_gender(p, gender); };
        auto is_prev_gender = [&](const std::pair<Entity *, int> &p, DemographicProfile::Gender gender) { return is_prevalent(p) && is_gender(p, gender); };
        auto is_incident_gender = [&](const std::pair<Entity *, int> &p, DemographicProfile::Gender gender) { return is_incident(p) && is_gender(p, gender); };
        auto is_sa_entity_type = [&](const std::pair<Entity *, int> &p, const std::string &entity_type) { return in_partition(p) && is_sexually_active(p) && is_entity_type(p, entity_type); };
        auto is_na_entity_type = [&](const std::pair<Entity *, int> &p, const std::string &entity_type) { return in_partition(p) && !is_sexually_active(p) && is_entity_type(p, entity_type); };
        auto is_prev_entity_type = [&](const std::pair<Entity *, int> &p, const std::string &entity_type) { return is_prevalent(p) && is_entity_type(p, entity_type); };
        auto is_incident_entity_type = [&](const std::pair<Entity *, int> &p, const std::string &entity_type) { return is_incident(p) && is_entity_type(p, entity_type); };
		auto is_sa_in_age_range_gender = [&](const std::pair<Entity *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return is_sa_gender(p, gender) && p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper; };
		auto is_prev_in_age_range_gender = [&](const std::pair<Entity *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper && is_prev_gender(p, gender); };
		auto is_incident_in_age_range_gender = [&](const std::pair<Entity *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return is_incident_gender(p, gender) && p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper; };
		auto is_sa_in_age_range_entity_type = [&](const std::pair<Entity *, int> &p, int lower, int upper, const std::string &entity_type) { return is_sa_entity_type(p, entity_type) && p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper; };
		auto is_prev_in_age_range_entity_type = [&](const std::pair<Entity *, int> &p, int lower, int upper, const std::string &entity_type) { return p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper && is_prev_entity_type(p, entity_type); };
		auto is_incident_in_age_range_entity_type = [&](const std::pair<Entity *, int> &p, int lower, int upper, const std::string &entity_type) { return is_incident_entity_type(p, entity_type) && p.first->getAge().in_months() >= lower && p.first->getAge().in_months() <= upper; };
        auto is_in_risk_group = [&](const std::pair<Entity *, int> &p, const std::string &risk_string)
        {
            if(!in_partition(p)) return false;

            if(risk_string == "CSW High Risk")
            {
                return p.first->getRiskLevel() == Entity::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::Csw;
            }
            if(risk_string == "CSW Low Risk")
            {
                return p.first->getRiskLevel() == Entity::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::Csw;
            }
            if(risk_string == "Non-CSW High Risk Male:Hetero")
            {
                return p.first->getEntityType() == "male" && p.first->getRiskLevel() == Entity::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW High Risk Male:Msmw")
            {
                return p.first->getEntityType() == "msmw" && p.first->getRiskLevel() == Entity::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW High Risk Male:Msm")
            {
                return p.first->getEntityType() == "msm" && p.first->getRiskLevel() == Entity::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW High Risk Female")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female && p.first->getRiskLevel() == Entity::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Male:Hetero")
            {
                return p.first->getEntityType() == "male" && p.first->getRiskLevel() == Entity::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Male:Msmw")
            {
                return p.first->getEntityType() == "msmw" && p.first->getRiskLevel() == Entity::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Male:Msm")
            {
                return p.first->getEntityType() == "msm" && p.first->getRiskLevel() == Entity::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Female")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female && p.first->getRiskLevel() == Entity::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            throw std::runtime_error("unknown risk group");
        };

        auto is_prev_in_risk_group = [&](const std::pair<Entity *, int> &p, const std::string &risk_string) { return is_prevalent(p) && is_in_risk_group(p, risk_string); };
        auto is_incident_in_risk_group = [&](const std::pair<Entity *, int> &p, const std::string &risk_string) { return is_incident(p) && is_in_risk_group(p, risk_string); };

        for(auto &partition : partitions_)
        {
            if(partition.GetLabel() == partition_name)
            {
                PartitionSummary summary;

                summary.population_size = std::count_if(member_partitions_.begin(), member_partitions_.end(), in_partition);
                summary.population_size_sa = std::count_if(member_partitions_.begin(), member_partitions_.end(), is_sexually_active);
                summary.population_size_na = std::count_if(member_partitions_.begin(), member_partitions_.end(), not_sexually_active);
                summary.incident_cases = std::count_if(member_partitions_.begin(), member_partitions_.end(), is_incident);
                summary.prevalent_cases = std::count_if(member_partitions_.begin(), member_partitions_.end(), is_prevalent);

                summary.pop_size_sa_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.prevalent_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.incident_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.pop_size_na_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_na_gender, std::placeholders::_1, DemographicProfile::Gender::Male));

                for(auto entity_type : {"male", "msmw", "msm", "female"})
                {
                    summary.pop_size_sa_entity_type[entity_type] = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_entity_type, std::placeholders::_1, entity_type));
                    summary.prevalent_entity_type[entity_type] = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_entity_type, std::placeholders::_1, entity_type));
                    summary.incident_entity_type[entity_type] = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_entity_type, std::placeholders::_1, entity_type));
                    summary.pop_size_na_entity_type[entity_type] = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_na_entity_type, std::placeholders::_1, entity_type));
                }

                std::vector<std::pair<int, int>> age_ranges = {{0, 203}, {204, 239}, {240, 299}, {300, 359}, {360, 419}, {420, 479}, {480, 539}, {540, 599}, {600, 1211}};

                for(auto &age_range : age_ranges)
                {
                    summary.sa_size_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));
                    summary.prevalent_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));
                    summary.incident_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));

                    for(auto entity_type : {"male", "msmw", "msm", "female"})
                    {
                        summary.sa_size_by_age_range_entity_type[entity_type].push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_in_age_range_entity_type, std::placeholders::_1, age_range.first, age_range.second, entity_type))));
                        summary.prevalent_by_age_range_entity_type[entity_type].push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_in_age_range_entity_type, std::placeholders::_1, age_range.first, age_range.second, entity_type))));
                        summary.incident_by_age_range_entity_type[entity_type].push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_in_age_range_entity_type, std::placeholders::_1, age_range.first, age_range.second, entity_type))));
                    }
                }

                for(auto risk_group : {"CSW High Risk", "CSW Low Risk", "Non-CSW High Risk Male:Hetero", "Non-CSW High Risk Male:Msmw", "Non-CSW High Risk Male:Msm", "Non-CSW High Risk Female", "Non-CSW Low Risk Male:Hetero", "Non-CSW Low Risk Male:Msmw", "Non-CSW Low Risk Male:Msm", "Non-CSW Low Risk Female"})
                {
                    summary.size_risk_group.push_back({risk_group, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_in_risk_group, std::placeholders::_1, risk_group))});
                    summary.prevalent_risk_group.push_back({risk_group, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_in_risk_group, std::placeholders::_1, risk_group))});
                    summary.incident_risk_group.push_back({risk_group, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_in_risk_group, std::placeholders::_1, risk_group))});
                }

				summary.life_months_undiscounted = 0;
				summary.life_months_discounted = 0;
				summary.cepac_costs_undiscounted = 0;
				summary.cepac_costs_discounted = 0;
				summary.cdm_costs_undiscounted = 0;
				summary.cdm_costs_discounted = 0;

				for (auto person : member_partitions_)
				{
					if (person.second == static_cast<int>(partition_index))
					{
						summary.life_months_undiscounted++;

                        auto discount = parameters.useRollout ?
                            parameters.untreatedContext->getRunSpecsInputs()->discountFactor
                            : parameters.cepacSimContexts.front()->getRunSpecsInputs()->discountFactor;
						summary.cepac_costs_undiscounted += person.first->get_monthly_cepac_costs_undiscounted();
						summary.cepac_costs_discounted += person.first->get_monthly_cepac_costs_discounted();
						summary.cdm_costs_undiscounted += person.first->get_monthly_cdm_costs_undiscounted();
						summary.cdm_costs_discounted += person.first->get_monthly_cdm_costs_discounted();
					}
				}

                return summary;
            }
        }

        throw std::runtime_error("partition not found");
    }

    std::string GetLabel() const { return label_; }

    void Remove(Entity *p) { if(member_partitions_.find(p) != member_partitions_.end()) member_partitions_.erase(p); }

private:
    struct
    {
        Time start;
        Time end;
    } enrollment_period_;

    bool open_;
    std::string label_;

    class Partition
    {
    public:
        double GetProportion() const { return proportion_; }

        void ApplyInterventions(Time current_time, Population &pop, Entity *p)
        {
            for(auto &intervention : interventions_)
            {
	        intervention.Apply(current_time, pop, p);
            }
        }

        void Update(Population &p, int current_time);
        std::string GetLabel() const { return label_; }
    private:
        friend class TargetGroup;
        std::string label_;
        bool trace_;
        double proportion_;
        std::vector<Intervention> interventions_;
    };

    std::vector<Partition> partitions_;

    std::unordered_map<Entity *, int> member_partitions_;

    Nullable<PopulationTarget> target_;
};

} // namespace transm
