#ifndef PREPELIGIBILITY_HPP
#define PREPELIGIBILITY_HPP

#include <cstddef>
#include <vector>

#include "entitytypes.hpp"

namespace transm {

/** PrEP starts in simulation year 57 (Entity::updatePrepStatus). */
constexpr int PrepFirstYear = 57;

/**
 * Who may start PrEP, from <prepEligibility>. A person must be undiagnosed, sexually
 * active and partnered, have at least currentPartnerCount partnerships when it is
 * given, and meet at least one of the risk criteria given (partner status, partner
 * risk level, recent condomless act). A criterion left out is not applied; with no
 * risk criteria, nobody is excluded by them. They decide who may start or restart
 * PrEP; people already on PrEP stay on it while they remain partnered. */
struct PrepEligibility
{
    /** <currentPartnerCount> or <partnerCount>: at least this many current partnerships; -1 = not applied */
    int currentPartnerCount = -1;

    /** <partnerStatus>: at least one current partner with this HIV status */
    bool checkPartnerStatus = false;
    HIVStatus partnerStatus = HIVStatus::ANY_POSITIVE;

    /** <partnerRiskLevel>: at least one current partner at this risk level */
    bool checkPartnerRiskLevel = false;
    RiskLevel partnerRiskLevel = RiskLevel::HIGH;

    /** <monthsSinceUnprotectedAct>: a condomless act within this many months; -1 = not applied */
    int monthsSinceUnprotectedAct = -1;

    bool TracksUnprotectedActs() const { return monthsSinceUnprotectedAct >= 0; }
};

/** What the eligibility rule needs to know about one current partner. */
struct PrepPartner
{
    HIVStatus status;
    RiskLevel risk;
};

/**
 * Whether a person's HIV status matches a <partnerStatus> value. The group values
 * mean: any-positive = infected, diagnosed or not; observed-positive = diagnosed;
 * not-observed-positive = infected but undiagnosed. Any other value must match exactly. */
inline bool HivStatusMatches(HIVStatus wanted, HIVStatus actual)
{
    const bool observed = actual == HIVStatus::OBSERVED_ACUTE || actual == HIVStatus::OBSERVED_CHRONIC
                          || actual == HIVStatus::OBSERVED_LATESTAGE;
    const bool unobserved = actual == HIVStatus::UNOBSERVED_ACUTE || actual == HIVStatus::UNOBSERVED_CHRONIC
                            || actual == HIVStatus::UNOBSERVED_LATESTAGE;
    switch (wanted) {
    case HIVStatus::ANY_POSITIVE:              return observed || unobserved;
    case HIVStatus::ANY_OBSERVED_POSITIVE:     return observed;
    case HIVStatus::ANY_NOT_OBSERVED_POSITIVE: return unobserved;
    default:                                   return actual == wanted;
    }
}

/**
 * Whether the <prepEligibility> criteria hold for a person.
 *
 * @param partners the person's current partners
 * @param monthsSinceUnprotectedAct months since their last condomless act, or -1 if
 *        they have none on record */
inline bool MeetsPrepEligibility(const PrepEligibility &rule, const std::vector<PrepPartner> &partners,
                                 int monthsSinceUnprotectedAct)
{
    if (rule.currentPartnerCount >= 0 && (int)partners.size() < rule.currentPartnerCount)
        return false;

    if (!rule.checkPartnerStatus && !rule.checkPartnerRiskLevel && !rule.TracksUnprotectedActs())
        return true;

    for (const auto &partner : partners) {
        if (rule.checkPartnerStatus && HivStatusMatches(rule.partnerStatus, partner.status))
            return true;
        if (rule.checkPartnerRiskLevel && partner.risk == rule.partnerRiskLevel)
            return true;
    }

    return rule.TracksUnprotectedActs() && monthsSinceUnprotectedAct >= 0
           && monthsSinceUnprotectedAct <= rule.monthsSinceUnprotectedAct;
}

/**
 * A yearly rate from a list that starts in PrepFirstYear; years past its end use the
 * last value, as does every year before PrepFirstYear (PrEP is off then anyway). */
inline double PrepRateForYear(const std::vector<double> &rates, int year)
{
    std::size_t index = year >= PrepFirstYear && year <= 102 ? (std::size_t)(year - PrepFirstYear) : 46;
    if (index >= rates.size())
        index = rates.size() - 1;
    return rates[index];
}

} // namespace transm

#endif /* PREPELIGIBILITY_HPP */
