#include "monthlystats.hpp"

namespace transm {

MonthlyStats::MonthlyStats() 
{
}

MonthlyStats::~MonthlyStats() 
{
}

void MonthlyStats::RecordEntity(int /*month*/, const Entity * /*entity*/)
{
}

void MonthlyStats::RecordRiskGroupChanged(int /*month*/, const Entity * /*entity*/)
{
}

void MonthlyStats::RecordDeath(int /*month*/, Entity::DeathStatus /*cause_of_death*/)
{
}

std::vector<std::string> MonthlyStats::BuildMonthSummary(int /*month*/) const
{
    return {};
}

} // namespace transm
