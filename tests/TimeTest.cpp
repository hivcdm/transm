#include <gtest/gtest.h>
#include "utility/time.hpp"

using namespace transm;

// Test cases for TimeSpan
TEST(TimeSpanTest, ConstructorAndAccessors) {
    TimeSpan oneYearTenMonths(1, 10);
    EXPECT_EQ(oneYearTenMonths.get_years(), 1);
    EXPECT_EQ(oneYearTenMonths.get_months(), 10);
    EXPECT_EQ(oneYearTenMonths.in_months(), 22);
    EXPECT_NEAR(oneYearTenMonths.in_years(), 1.833, 0.001);

    TimeSpan twoYearsFourteenMonths(2, 14);
    EXPECT_EQ(twoYearsFourteenMonths.get_years(), 3);
    EXPECT_EQ(twoYearsFourteenMonths.get_months(), 2);
    EXPECT_EQ(twoYearsFourteenMonths.in_months(), 38);
    EXPECT_NEAR(twoYearsFourteenMonths.in_years(), 3.167, 0.001);

    TimeSpan threeYearsTwelveMonth(3, 12);
    EXPECT_EQ(threeYearsTwelveMonth.get_years(), 4);
    EXPECT_EQ(threeYearsTwelveMonth.get_months(), 0);
    EXPECT_EQ(threeYearsTwelveMonth.in_months(), 48);
    EXPECT_EQ(threeYearsTwelveMonth.in_years(), 4.0);
}

TEST(TimeSpanTest, ArithmeticOperations) {
    TimeSpan twoYears(2, 0);
    TimeSpan sixMonths(0, 6);
    auto result = twoYears + sixMonths;
    EXPECT_EQ(result.get_years(), 2);
    EXPECT_EQ(result.get_months(), 6);

    result -= sixMonths;
    EXPECT_EQ(result, twoYears);

    result = twoYears - sixMonths;
    EXPECT_EQ(result.get_years(), 1);
    EXPECT_EQ(result.get_months(), 6);
}

TEST(TimeSpanTest, ComparisonOperations) {
    TimeSpan oneYear(1, 0);
    TimeSpan twelveMonths(0, 12);
    EXPECT_EQ(oneYear, twelveMonths);
    EXPECT_LE(oneYear, twelveMonths);
    EXPECT_GE(oneYear, twelveMonths);
}

// Test cases for Time
TEST(TimeTest, ConstructorAndAccessors) {
    Time jan2021(2021, 1);
    EXPECT_EQ(jan2021.get_year(), 2021);
    EXPECT_EQ(jan2021.get_month(), 1);
    EXPECT_EQ(jan2021.in_months(), 24253); // Assuming the calculation based on the example
}

TEST(TimeTest, ArithmeticOperationsWithTimeSpan) {
    Time start(2021, 6);
    TimeSpan sixMonths(0, 6);
    auto future = start + sixMonths;
    EXPECT_EQ(future.get_year(), 2021);
    EXPECT_EQ(future.get_month(), 12);

    future -= sixMonths;
    EXPECT_EQ(future.get_year(), 2021);
    EXPECT_EQ(future.get_month(), 6);

    auto duration = future - start;
    EXPECT_EQ(duration.in_months(), 0);
}

TEST(TimeTest, ComparisonOperations) {
    Time jan2021(2021, 1);
    Time dec2021(2021, 12);
    EXPECT_LT(jan2021, dec2021);
    EXPECT_LE(jan2021, dec2021);
    EXPECT_GT(dec2021, jan2021);
    EXPECT_GE(dec2021, jan2021);
}



