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
        int population_size;
        int incident_cases;
        int prevalent_cases;
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
        bool include_non_sexually_active = false) const
    {
        int partition_index = 0;

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

        auto in_partition = [=](const std::pair<Person *, int> &p) { return p.second == partition_index; };
        auto is_sexually_active = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SA; };
        auto is_prevalent = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->ageInfected > -1 && p.first->ageInfected + 1 != (int)p.first->age; };
        auto is_incident = [&](const std::pair<Person *, int> &p) { return in_partition(p) && p.first->ageInfected + 1 == (int)p.first->age; };
        auto is_prevalent_sa = [&](const std::pair<Person *, int> &p) { return in_partition(p) && is_sexually_active(p) && is_prevalent(p); };
        auto is_incident_sa = [&](const std::pair<Person *, int> &p) { return in_partition(p) && is_sexually_active(p) && is_incident(p); };

        for(auto &partition : partitions_)
        {
            if(partition.GetLabel() == partition_name)
            {
                PartitionSummary summary;

                if(include_non_sexually_active)
                {
                    summary.population_size = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), in_partition);
                    summary.incident_cases = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), is_incident);
                    summary.prevalent_cases = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), is_prevalent);
                }
                else
                {
                    summary.population_size = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), is_sexually_active);
                    summary.incident_cases = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), is_incident_sa);
                    summary.prevalent_cases = (int)std::count_if(member_partitions_.begin(), member_partitions_.end(), is_prevalent_sa);
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