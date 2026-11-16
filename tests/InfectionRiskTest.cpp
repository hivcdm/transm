#include <gtest/gtest.h>

#include "entities/infectionrisk.hpp"

namespace transm {

TEST(InfectionRiskTest, PrepKeepsTheShareOfEfficacyEachAdherenceLevelAllows)
{
    EXPECT_DOUBLE_EQ(PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_ADHERENT), 0.8 * 0.99);
    EXPECT_DOUBLE_EQ(PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_SUBSTANTIALLY_ADHERENT), 0.8 * 0.80);
    EXPECT_DOUBLE_EQ(PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_PARTIALLY_ADHERENT), 0.8 * 0.10);
    EXPECT_DOUBLE_EQ(PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_INADHERENT), 0.8 * 0.05);
    EXPECT_EQ(PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_OFF_PREP), 0.0);
}

TEST(InfectionRiskTest, BetterAdherenceNeverProtectsLess)
{
    const PrepAherenceLevel worstToBest[] = {
        PrepAherenceLevel::PREP_OFF_PREP, PrepAherenceLevel::PREP_INADHERENT,
        PrepAherenceLevel::PREP_PARTIALLY_ADHERENT, PrepAherenceLevel::PREP_SUBSTANTIALLY_ADHERENT,
        PrepAherenceLevel::PREP_ADHERENT};
    for (double efficacy : {0.0, 0.5, 0.8, 1.0})
        for (int i = 1; i < 5; i++)
            EXPECT_LE(PrepEfficacyForAdherence(efficacy, worstToBest[i - 1]),
                      PrepEfficacyForAdherence(efficacy, worstToBest[i]))
                << "efficacy " << efficacy << ", level " << i;
}

TEST(InfectionRiskTest, WithoutProtectionTheRiskIsTheCoefficient)
{
    EXPECT_EQ(ForceOfInfection(0.0088, 0, 0, 0), 0.0088);
    EXPECT_EQ(ForceOfInfection(0, 0.5, 0.5, 0.5), 0.0);
}

/* Each protection must lower the risk on its own; dropping one from the product
 * (as a bug that ignored PrEP would) fails here. */
TEST(InfectionRiskTest, EachProtectionScalesTheRiskIndependently)
{
    const double coefficient = 0.065;
    EXPECT_DOUBLE_EQ(ForceOfInfection(coefficient, 0.8, 0, 0), coefficient * 0.2);
    EXPECT_DOUBLE_EQ(ForceOfInfection(coefficient, 0, 0.6, 0), coefficient * 0.4);
    EXPECT_DOUBLE_EQ(ForceOfInfection(coefficient, 0, 0, 0.792), coefficient * 0.208);
    EXPECT_DOUBLE_EQ(ForceOfInfection(coefficient, 0.8, 0.6, 0.792), coefficient * 0.2 * 0.4 * 0.208);
    EXPECT_EQ(ForceOfInfection(coefficient, 1, 0, 0), 0.0);
    EXPECT_EQ(ForceOfInfection(coefficient, 0, 0, 1), 0.0);
}

/* The two together, as Male::getFOI and Female::getFOI use them: an adherent user
 * of 80%-effective PrEP keeps 0.8 * 0.99 of the protection. */
TEST(InfectionRiskTest, AdherentPrepUserFacesAFifthOfTheRisk)
{
    const double coefficient = 0.018;
    const double prep = PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_ADHERENT);
    EXPECT_DOUBLE_EQ(ForceOfInfection(coefficient, 0, 0, prep), coefficient * (1 - 0.792));
    EXPECT_EQ(ForceOfInfection(coefficient, 0, 0, PrepEfficacyForAdherence(0.8, PrepAherenceLevel::PREP_OFF_PREP)),
              coefficient);
}

} // namespace transm
