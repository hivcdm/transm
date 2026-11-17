#include <gtest/gtest.h>

#include <vector>

#include "entities/prepeligibility.hpp"

namespace transm {

namespace {

const PrepPartner Negative{HIVStatus::NEGATIVE, RiskLevel::LOW};
const PrepPartner NegativeHighRisk{HIVStatus::NEGATIVE, RiskLevel::HIGH};
const PrepPartner Undiagnosed{HIVStatus::UNOBSERVED_CHRONIC, RiskLevel::LOW};
const PrepPartner Diagnosed{HIVStatus::OBSERVED_CHRONIC, RiskLevel::LOW};
const PrepPartner DiagnosedHighRisk{HIVStatus::OBSERVED_ACUTE, RiskLevel::HIGH};

/* The Miami scenario's <prepEligibility>. */
PrepEligibility MiamiRule()
{
    PrepEligibility rule;
    rule.currentPartnerCount = 1;
    rule.checkPartnerStatus = true;
    rule.partnerStatus = HIVStatus::ANY_POSITIVE;
    rule.checkPartnerRiskLevel = true;
    rule.partnerRiskLevel = RiskLevel::HIGH;
    rule.monthsSinceUnprotectedAct = 12;
    return rule;
}

} // namespace

TEST(PrepEligibilityTest, GroupStatusesMatchTheirMembers)
{
    for (HIVStatus s : {HIVStatus::OBSERVED_ACUTE, HIVStatus::OBSERVED_CHRONIC, HIVStatus::OBSERVED_LATESTAGE}) {
        EXPECT_TRUE(HivStatusMatches(HIVStatus::ANY_POSITIVE, s));
        EXPECT_TRUE(HivStatusMatches(HIVStatus::ANY_OBSERVED_POSITIVE, s));
        EXPECT_FALSE(HivStatusMatches(HIVStatus::ANY_NOT_OBSERVED_POSITIVE, s));
    }
    for (HIVStatus s : {HIVStatus::UNOBSERVED_ACUTE, HIVStatus::UNOBSERVED_CHRONIC, HIVStatus::UNOBSERVED_LATESTAGE}) {
        EXPECT_TRUE(HivStatusMatches(HIVStatus::ANY_POSITIVE, s));
        EXPECT_FALSE(HivStatusMatches(HIVStatus::ANY_OBSERVED_POSITIVE, s));
        EXPECT_TRUE(HivStatusMatches(HIVStatus::ANY_NOT_OBSERVED_POSITIVE, s));
    }
    for (HIVStatus group : {HIVStatus::ANY_POSITIVE, HIVStatus::ANY_OBSERVED_POSITIVE, HIVStatus::ANY_NOT_OBSERVED_POSITIVE})
        EXPECT_FALSE(HivStatusMatches(group, HIVStatus::NEGATIVE));
    EXPECT_TRUE(HivStatusMatches(HIVStatus::NEGATIVE, HIVStatus::NEGATIVE));
    EXPECT_FALSE(HivStatusMatches(HIVStatus::OBSERVED_CHRONIC, HIVStatus::OBSERVED_ACUTE));
}

TEST(PrepEligibilityTest, NoCriteriaMeansEveryoneQualifies)
{
    PrepEligibility none;
    EXPECT_TRUE(MeetsPrepEligibility(none, {}, -1));
    EXPECT_TRUE(MeetsPrepEligibility(none, {Negative}, -1));
}

TEST(PrepEligibilityTest, EachCriterionAloneDecides)
{
    PrepEligibility count;
    count.currentPartnerCount = 2;
    EXPECT_FALSE(MeetsPrepEligibility(count, {Negative}, -1));
    EXPECT_TRUE(MeetsPrepEligibility(count, {Negative, Negative}, -1));

    PrepEligibility status;
    status.checkPartnerStatus = true;
    status.partnerStatus = HIVStatus::ANY_OBSERVED_POSITIVE;
    EXPECT_FALSE(MeetsPrepEligibility(status, {Negative, Undiagnosed}, -1));
    EXPECT_TRUE(MeetsPrepEligibility(status, {Negative, Diagnosed}, -1));

    PrepEligibility risk;
    risk.checkPartnerRiskLevel = true;
    risk.partnerRiskLevel = RiskLevel::HIGH;
    EXPECT_FALSE(MeetsPrepEligibility(risk, {Negative, Diagnosed}, -1));
    EXPECT_TRUE(MeetsPrepEligibility(risk, {Negative, NegativeHighRisk}, -1));

    PrepEligibility recent;
    recent.monthsSinceUnprotectedAct = 12;
    EXPECT_FALSE(MeetsPrepEligibility(recent, {Negative}, -1)) << "no condomless act on record";
    EXPECT_TRUE(MeetsPrepEligibility(recent, {Negative}, 0));
    EXPECT_TRUE(MeetsPrepEligibility(recent, {Negative}, 12));
    EXPECT_FALSE(MeetsPrepEligibility(recent, {Negative}, 13));
}

/* The partner count is required; then any one risk criterion is enough. */
TEST(PrepEligibilityTest, MiamiRuleNeedsAPartnerAndAnyOneRiskCriterion)
{
    const PrepEligibility rule = MiamiRule();
    EXPECT_TRUE(MeetsPrepEligibility(rule, {Diagnosed}, -1)) << "positive partner";
    EXPECT_TRUE(MeetsPrepEligibility(rule, {Undiagnosed}, -1)) << "positive partner, undiagnosed";
    EXPECT_TRUE(MeetsPrepEligibility(rule, {NegativeHighRisk}, -1)) << "high-risk partner";
    EXPECT_TRUE(MeetsPrepEligibility(rule, {Negative}, 12)) << "condomless act within 12 months";
    EXPECT_TRUE(MeetsPrepEligibility(rule, {DiagnosedHighRisk}, 3)) << "all three";
    EXPECT_FALSE(MeetsPrepEligibility(rule, {Negative}, 13)) << "none: last condomless act too long ago";
    EXPECT_FALSE(MeetsPrepEligibility(rule, {Negative}, -1)) << "none: never a condomless act";
    EXPECT_FALSE(MeetsPrepEligibility(rule, {}, 3)) << "the partner count is required";

    PrepEligibility twoPartners = rule;
    twoPartners.currentPartnerCount = 2;
    EXPECT_FALSE(MeetsPrepEligibility(twoPartners, {DiagnosedHighRisk}, 3));
    EXPECT_TRUE(MeetsPrepEligibility(twoPartners, {Negative, NegativeHighRisk}, -1));
}

/* The yearly lists start in year 57, PrEP's first year; later years use the last value. */
TEST(PrepEligibilityTest, YearlyRatesStartInThePrepYearAndKeepTheLastValue)
{
    const std::vector<double> rates = {0.1, 0.2, 0.3};
    EXPECT_EQ(PrepRateForYear(rates, 57), 0.1);
    EXPECT_EQ(PrepRateForYear(rates, 58), 0.2);
    EXPECT_EQ(PrepRateForYear(rates, 59), 0.3);
    EXPECT_EQ(PrepRateForYear(rates, 70), 0.3);
    EXPECT_EQ(PrepRateForYear(rates, 200), 0.3);
    EXPECT_EQ(PrepRateForYear({0.5}, 60), 0.5);
}

} // namespace transm
