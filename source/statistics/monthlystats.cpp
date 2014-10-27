#include "monthlystats.hpp"

namespace transm {

void MonthlyStats::RecordEntity(int /*month*/, const Entity * /*entity*/)
{
}

void MonthlyStats::RecordRiskGroupChanged(int /*month*/, const Entity * /*entity*/)
{
}

void MonthlyStats::RecordDeath(int /*month*/, Entity::DeathStatus /*cause_of_death*/)
{
}

} // namespace transm
