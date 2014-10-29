#include <numeric>
#include <set>
#include <xlnt/xlnt.hpp>

#include "populationstatistics.hpp"
#include "entities/male.hpp"

namespace transm {

PopulationStatistics::MonthStats &PopulationStatistics::GetMonthStats(int month, bool create)
{
    if(month >= (int)stats_.size())
    {
        if(create && month == (int)stats_.size())
        {
            stats_.push_back(MonthStats());
        }
        else
        {
            throw std::runtime_error("stats not found for month " + std::to_string(month));
        }
    }

    return stats_.at(month);
}

void PopulationStatistics::RecordEntity(int month, const Entity *entity)
{
    auto &month_stats = GetMonthStats(month, true);

    auto age_group = AgeGroup{0, 10};
    auto sexual_activity_status = entity->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>();
    auto gender = entity->getDemographicProfileVal<DemographicProfile::Gender>();
    auto risk = entity->getRiskLevel();
    auto employment = entity->getDemographicProfileVal<DemographicProfile::Employment>();
    auto bucket = std::make_tuple(age_group, sexual_activity_status, gender, risk, employment);
    month_stats.pop_size[bucket]++;

    if(gender == DemographicProfile::Gender::Male
        && ((Male *)entity)->IsCircumcised())
    {
        month_stats.num_circumcised[sexual_activity_status]++;
    }
}

void PopulationStatistics::RecordDeath(int month, Entity::DeathStatus cause_of_death)
{
    auto &month_stats = GetMonthStats(month, true);
    month_stats.death_cause_count[cause_of_death]++;
}

std::vector<std::string> PopulationStatistics::BuildMonthSummary(int m) const
{
    const auto &month = stats_.at(m);
    std::vector<std::string> formatted;

    // Pop Size
    formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL, 
        [](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
    {
        return sum + i.second;
    })));

    // SA Pop Size
    formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
        [](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
    {
        return sum + (std::get<1>(i.first) == DemographicProfile::SexualActivityStatus::Active ? i.second : 0);
    })));

    //Deaths
    std::size_t total_deaths = 0;
    for(auto death_cause : enum_iterator<Entity::DeathStatus>())
    {
        auto deaths = std::accumulate(month.death_cause_count.begin(), month.death_cause_count.end(), 0ULL,
            [=](std::size_t sum, const DeathCauseCount::value_type &i)
        {
            return sum + (i.first == death_cause ? i.second : 0);
        });
        formatted.push_back(std::to_string(deaths));
        total_deaths += deaths;
    }
    // Total (Deaths)
    formatted.push_back(std::to_string(total_deaths));

    // By Gender
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
        auto num_gender = std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
            [=](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
        {
            return sum + (std::get<2>(i.first) == gender ? i.second : 0);
        });
        formatted.push_back(std::to_string(num_gender));
    }

    // By Risk Group
    for(auto employment : enum_iterator<DemographicProfile::Employment>())
    {
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
        {
            if(employment == DemographicProfile::Employment::Csw 
                && gender != DemographicProfile::Gender::Female)
            {
                continue; // skip non-female csws
            }

            for(auto risk : enum_iterator<Entity::RiskLevel>())
            {
                auto num_risk_group = std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
                    [=](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
                {
                    return sum + ((std::get<2>(i.first) == gender 
                        && std::get<3>(i.first) == risk 
                        && std::get<4>(i.first) == employment) ? i.second : 0);
                });
                formatted.push_back(std::to_string(num_risk_group));
            }
        }
    }

    // Non-SA Pop (All Ages)
    formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
        [&](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
    {
        return sum + (std::get<1>(i.first) == DemographicProfile::SexualActivityStatus::NotActive ? i.second : 0);
    })));

    assert(!age_groups_.empty());

    // SA Pop (Age Months)
    for(const auto &age_group : age_groups_)
    {
        formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
            [&](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
        {
            return sum + (std::get<0>(i.first) == age_group 
                && std::get<1>(i.first) == DemographicProfile::SexualActivityStatus::Active ? i.second : 0);
        })));
    }

    // Gender (By Age)
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
        // Non-SA
        formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
            [&](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
        {
            return sum + (std::get<1>(i.first) == DemographicProfile::SexualActivityStatus::NotActive 
                && std::get<2>(i.first) == gender ? i.second : 0);
        })));

        // SA Pop (Age Months)
        for(const auto &age_group : age_groups_)
        {
            formatted.push_back(std::to_string(std::accumulate(month.pop_size.begin(), month.pop_size.end(), 0ULL,
                [=](std::size_t sum, const AgeActivityGenderRiskEmplCount::value_type &i)
            {
                return sum + (std::get<0>(i.first) == age_group
                    && std::get<1>(i.first) == DemographicProfile::SexualActivityStatus::Active
                    && std::get<2>(i.first) == gender ? i.second : 0);
            })));
        }
    }

    // Number circumcised by sexual activity status
    for(auto sexual_activity_status : enum_iterator<DemographicProfile::SexualActivityStatus>())
    {
        formatted.push_back(std::to_string(std::accumulate(month.num_circumcised.begin(), month.num_circumcised.end(), 0ULL,
            [=](std::size_t sum, const std::unordered_map<DemographicProfile::SexualActivityStatus, std::size_t>::value_type &i)
        {
            return sum + (i.first == sexual_activity_status ? i.second : 0);
        })));
    }

    return formatted;
}

} // namespace transm
