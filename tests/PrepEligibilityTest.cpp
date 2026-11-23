#include <gtest/gtest.h>

#include <array>

#include "entities/prepeligibility.hpp"

namespace transm {

/* The lookup used positions 1 to 12 of the twelve values: the first was never used,
 * and from the 13th month it read past the end of the list. */
TEST(PrepEligibilityTest, StopChanceUsesEachMonthsValueAndKeepsTheTwelfth)
{
    const std::array<double, 12> chances = {0.14, 0.059, 0.061, 0.012, 0.012, 0.012,
                                            0.021, 0.021, 0.022, 0.004, 0.005, 0.006};
    for (int month = 1; month <= 12; month++)
        EXPECT_EQ(PrepStopChance(chances, month), chances[month - 1]) << "month " << month;
    for (int month : {13, 14, 60, 500})
        EXPECT_EQ(PrepStopChance(chances, month), 0.006) << "month " << month;
    EXPECT_EQ(PrepStopChance(chances, 0), 0.14);
}

} // namespace transm
