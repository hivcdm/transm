#ifndef POPULATIONSTATISTICS_HPP
#define POPULATIONSTATISTICS_HPP

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
    /*virtual*/ void RecordEntity(int month, const Entity *entity) override;
    /*virtual*/ void RecordDeath(int month, DeathStatus cause_of_death) override;
    /*virtual*/ std::vector<std::string> BuildMonthSummary(int month) const override;

protected:
    /*virtual*/ MonthStats &GetMonthStats(int month, bool create);

private:
    std::vector<MonthStats> stats_;
};

} // namespace transm

#endif /* POPULATIONSTATISTICS_HPP */