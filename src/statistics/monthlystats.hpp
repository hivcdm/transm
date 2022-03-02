#ifndef MONTHLYSTATS_HPP
#define MONTHLYSTATS_HPP

#include <tuple>
#include <unordered_map>

#include "entities/entity.hpp"
#include "utility/hash_tuple.hpp"

namespace xlnt {
class worksheet;
} // namespace xlnt

namespace std {
template<>
struct hash<transm::DeathStatus>
{
    size_t operator()(const transm::DeathStatus &g) const
    {
      return hash<std::size_t>()(static_cast<std::size_t>(g));
    }
};

template<>
struct hash<transm::DemographicProfile::SexualActivityStatus>
{
  size_t operator()(const transm::DemographicProfile::SexualActivityStatus &g) const
    {
      return hash<std::size_t>()(static_cast<std::size_t>(g));
    }
};

template<>
struct hash<transm::DemographicProfile::Gender>
{
  size_t operator()(const transm::DemographicProfile::Gender &g) const
    {
      return hash<std::size_t>()(static_cast<std::size_t>(g));
    }
};

template<>
struct hash<transm::DemographicProfile::Employment>
{
  size_t operator()(const transm::DemographicProfile::Employment &g) const
    {
      return hash<std::size_t>()(static_cast<std::size_t>(g));
    }
};

template<>
struct hash<transm::RiskLevel>
{
  size_t operator()(const transm::RiskLevel &g) const
    {
      return hash<std::size_t>()(static_cast<std::size_t>(g));
    }
};
} /* namespace std */

namespace transm {
using DeathCauseCount = std::unordered_map<DeathStatus, std::size_t>;
struct AgeGroup
{
    bool operator==(const AgeGroup &other) const { return other.lower == lower && other.upper == upper; }
    int lower;
    int upper;
};
using AgeActivityGenderRiskEmpl = std::tuple<AgeGroup, DemographicProfile::SexualActivityStatus, DemographicProfile::Gender, RiskLevel, DemographicProfile::Employment>;
} /* namespace transm */

namespace std {
template<>
struct hash<transm::AgeGroup>
{
    size_t operator()(const transm::AgeGroup &g) const
    {
        size_t seed = 0;
        hash_combine(seed, g.lower);
        hash_combine(seed, g.upper);
        return seed;
    }
};
} // namespace std

namespace transm {

using AgeActivityGenderRiskEmplCount = std::unordered_map<AgeActivityGenderRiskEmpl, std::size_t>;

class MonthlyStats
{
public:
    MonthlyStats();
    virtual ~MonthlyStats();

    void SetAgeGroups(const std::vector<AgeGroup> &age_groups) { age_groups_ = age_groups; }

    virtual void RecordEntity(int month, const Entity *entity);
    virtual void RecordRiskGroupChanged(int month, const Entity *entity);
    virtual void RecordDeath(int month, DeathStatus cause_of_death);

    virtual std::vector<std::string> BuildMonthSummary(int month) const;

protected:
    std::vector<AgeGroup> age_groups_;
};

} // namespace transm


#endif /* MONTHLYSTATS_HPP */