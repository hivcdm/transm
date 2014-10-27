#pragma once

#include "monthlystats.hpp"

namespace transm {

class PopulationStatistics : public MonthlyStats
{
    struct MonthStats
    {
        AgeActivityGenderRiskEmplCount pop_size;
        DeathCauseCount death_cause_count;
        std::unordered_map<DemographicProfile::SexualActivityStatus, std::size_t> num_circumcised;
    };

public:
    /*virtual*/ void RecordEntity(int month, const Entity *entity);
    /*virtual*/ void RecordDeath(int month, Entity::DeathStatus cause_of_death);
    /*virtual*/ std::vector<std::string> BuildMonthSummary(int month) const;

protected:
    /*virtual*/ MonthStats &GetMonthStats(int month, bool create);

private:
    std::vector<MonthStats> stats_;
};

} // namespace transm