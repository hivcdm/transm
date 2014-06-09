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

    TargetGroup(int start, int end, bool open, bool permanent, Nullable<PopulationTarget> target);

    void Update(Population &p, int simulation_time, RandomNumberGenerator &rng, const std::unordered_set<Person *> &new_people, const std::unordered_set<Person *> &dead_people);

    void AddPartition(const std::string &label, bool trace, double proportion,
        std::vector<Intervention> simulation_interventions);

private:
    struct
    {
        int start;
        int end;
    } enrollment_period_;

    bool open_;
    bool permanent_effect_;

    class Partition
    {
    public:
        double GetProportion() const { return proportion_; }
        void Add(Population &pop, Person *p) 
        { 
            members_.insert(p); 
            for(auto &intervention : interventions_)
            {
                intervention.Apply(pop, p);
            }
        }
        void Remove(Person *p) { if(members_.find(p) != members_.end()) members_.erase(p); }
        void Update(Population &p, int current_time);
    private:
        friend class TargetGroup;
        std::unordered_set<Person *> members_;
        std::string label_;
        bool trace_;
        double proportion_;
        std::vector<Intervention> interventions_;
    };

    std::vector<Partition> partitions_;

    Nullable<PopulationTarget> target_;
};