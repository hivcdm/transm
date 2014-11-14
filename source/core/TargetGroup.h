#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "../util/Nullable.h"
#include "../entities/Person.h"
#include "../entities/classifiers/DemographicProfile.h"

class TargetGroup
{
public:
    struct PopulationTarget
    {
        static PopulationTarget Any;

        Nullable<Person::RiskLevel> risk_level;
        Nullable<DemographicProfile::Employment> employment;
        Nullable<DemographicProfile::SexualActivityStatus> sexual_activity_status;
        Nullable<DemographicProfile::Gender> gender;
        Nullable<DemographicProfile::RelationshipStatus> relationship_status;
        Nullable<DemographicProfile::SexualOrientation> sexual_orientation;
        Nullable<int> age_lower;
        Nullable<int> age_upper;
        Nullable<Person::HIVStatus> observed_hiv_status;
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
        std::size_t pop_size_sa_female;
        std::size_t pop_size_na_male;
        std::size_t pop_size_na_female;
        std::vector<std::tuple<int, int, std::size_t>> sa_size_by_age_range_male;
        std::vector<std::tuple<int, int, std::size_t>> sa_size_by_age_range_female;
        std::vector<std::pair<std::string, std::size_t>> size_risk_group;
        std::size_t prevalent_male;
        std::size_t prevalent_female;
        std::vector<std::tuple<int, int, std::size_t>> prevalent_by_age_range_male;
        std::vector<std::tuple<int, int, std::size_t>> prevalent_by_age_range_female;
        std::vector<std::pair<std::string, std::size_t>> prevalent_risk_group;
        std::size_t incident_male;
        std::size_t incident_female;
        std::vector<std::tuple<int, int, std::size_t>> incident_by_age_range_male;
        std::vector<std::tuple<int, int, std::size_t>> incident_by_age_range_female;
        std::vector<std::pair<std::string, std::size_t>> incident_risk_group;
		double life_months_undiscounted;
		double life_months_discounted;
		double cdm_costs_undiscounted;
		double cdm_costs_discounted;
		double cepac_costs_undiscounted;
		double cepac_costs_discounted;

    };

    TargetGroup(const std::string &label, int start, int end, bool open, bool permanent, Nullable<PopulationTarget> target);

    void Update(Population &p, int simulation_time, RandomNumberGenerator &rng, 
        const std::unordered_set<Person *> &dead_people);

    void AddPartition(const std::string &label, bool trace, double proportion,
        std::vector<Intervention> simulation_interventions);

    void AssignToPartition(Population &pop, Person *p, int partition) 
    { 
        if(!InGroup(p))
        {
            member_partitions_[p] = partition;
            partitions_[partition].ApplyInterventions(pop, p);
        }
    }

    bool InGroup(Person *p) const { return member_partitions_.find(p) != member_partitions_.end(); }

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
        bool /*include_non_sexually_active*/ = true) const
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

        auto in_partition = [=](const std::pair<Person *, int> &p) { return p.second == (int)partition_index; };
        auto is_sexually_active = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::Active; };
        auto not_sexually_active = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::NotActive; };
        auto is_prevalent = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->ageInfected > -1 && p.first->ageInfected + 1 != (int)p.first->age; };
        auto is_incident = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->ageInfected + 1 == (int)p.first->age; };
//        auto is_prevalent_sa = [&](const std::pair<Person *, int> &p) { return in_partition(p) && is_sexually_active(p) && is_prevalent(p); };
//        auto is_incident_sa = [&](const std::pair<Person *, int> &p) { return in_partition(p) && is_sexually_active(p) && is_incident(p); };
        auto is_gender = [&](const std::pair<Person *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::Gender>() == gender; };
        auto is_sa_gender = [&](const std::pair<Person *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && is_sexually_active(p) && is_gender(p, gender); };
        auto is_na_gender = [&](const std::pair<Person *, int> &p, DemographicProfile::Gender gender) { return in_partition(p) && !is_sexually_active(p) && is_gender(p, gender); };
        auto is_prev_gender = [&](const std::pair<Person *, int> &p, DemographicProfile::Gender gender) { return is_prevalent(p) && is_gender(p, gender); };
        auto is_incident_gender = [&](const std::pair<Person *, int> &p, DemographicProfile::Gender gender) { return is_incident(p) && is_gender(p, gender); };
        auto is_sa_in_age_range_gender = [&](const std::pair<Person *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return is_sa_gender(p, gender) && p.first->getAge(TimeGranularity::Month) >= lower && p.first->getAge(TimeGranularity::Month) <= upper; };
        auto is_prev_in_age_range_gender = [&](const std::pair<Person *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return p.first->getAge(TimeGranularity::Month) >= lower && p.first->getAge(TimeGranularity::Month) <= upper && is_prev_gender(p, gender); };
        auto is_incident_in_age_range_gender = [&](const std::pair<Person *, int> &p, int lower, int upper, DemographicProfile::Gender gender) { return is_incident_gender(p, gender) && p.first->getAge(TimeGranularity::Month) >= lower && p.first->getAge(TimeGranularity::Month) <= upper; };
        auto is_in_risk_group = [&](const std::pair<Person *, int> &p, const std::string &risk_string)
        {
            if(!in_partition(p)) return false;

            if(risk_string == "CSW High Risk")
            {
                return p.first->getRiskLevel() == Person::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::Csw;
            }
            if(risk_string == "CSW Low Risk")
            {
                return p.first->getRiskLevel() == Person::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::Csw;
            }
            if(risk_string == "Non-CSW High Risk Male")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male && p.first->getRiskLevel() == Person::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW High Risk Female")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female && p.first->getRiskLevel() == Person::RiskLevel::HIGH && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Male")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male && p.first->getRiskLevel() == Person::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            if(risk_string == "Non-CSW Low Risk Female")
            {
                return p.first->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female && p.first->getRiskLevel() == Person::RiskLevel::LOW && p.first->getDemographicProfileVal<DemographicProfile::Employment>() == DemographicProfile::Employment::NonCsw;
            }
            throw std::runtime_error("unknown risk group");
        };
//        auto is_sa_in_risk_group = [&](const std::pair<Person *, int> &p, const std::string &risk_string) { return is_sexually_active(p) && is_in_risk_group(p, risk_string); };
        auto is_prev_in_risk_group = [&](const std::pair<Person *, int> &p, const std::string &risk_string) { return is_prevalent(p) && is_in_risk_group(p, risk_string); };
        auto is_incident_in_risk_group = [&](const std::pair<Person *, int> &p, const std::string &risk_string) { return is_incident(p) && is_in_risk_group(p, risk_string); };

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
                summary.pop_size_sa_female = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_gender, std::placeholders::_1, DemographicProfile::Gender::Female));
                summary.prevalent_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.prevalent_female = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_gender, std::placeholders::_1, DemographicProfile::Gender::Female));
                summary.incident_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.incident_female = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_gender, std::placeholders::_1, DemographicProfile::Gender::Female));

                summary.pop_size_na_male = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_na_gender, std::placeholders::_1, DemographicProfile::Gender::Male));
                summary.pop_size_na_female = std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_na_gender, std::placeholders::_1, DemographicProfile::Gender::Female));

                std::vector<std::pair<int, int>> age_ranges = {{0, 203}, {204, 239}, {240, 299}, {300, 359}, {360, 419}, {420, 479}, {480, 539}, {540, 599}, {600, 1211}};

                for(auto &age_range : age_ranges)
                {
                    summary.sa_size_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));
                    summary.prevalent_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));
                    summary.incident_by_age_range_male.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Male))));
                    summary.sa_size_by_age_range_female.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_sa_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Female))));
                    summary.prevalent_by_age_range_female.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_prev_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Female))));
                    summary.incident_by_age_range_female.push_back(std::make_tuple(age_range.first, age_range.second, (std::size_t)std::count_if(member_partitions_.begin(), member_partitions_.end(), std::bind(is_incident_in_age_range_gender, std::placeholders::_1, age_range.first, age_range.second, DemographicProfile::Gender::Female))));
                }

                for(auto risk_group : {"CSW High Risk", "CSW Low Risk", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female"})
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
					if (person.second == partition_index)
					{
						summary.life_months_undiscounted++;
						summary.life_months_discounted += person.first->getCepacDiscountFactor();
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

    void Remove(Person *p) { if(member_partitions_.find(p) != member_partitions_.end()) member_partitions_.erase(p); }

private:
    struct
    {
        int start;
        int end;
    } enrollment_period_;

    bool open_;
    bool permanent_effect_;
    std::string label_;

    class Partition
    {
    public:
        double GetProportion() const { return proportion_; }

        void ApplyInterventions(Population &pop, Person *p) 
        { 
            for(auto &intervention : interventions_)
            {
                intervention.Apply(pop, p);
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

    std::unordered_map<Person *, int> member_partitions_;

    Nullable<PopulationTarget> target_;
};
