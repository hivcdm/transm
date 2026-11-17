#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "utility/time.hpp"
#include "utility/runtimepredictor.hpp"

namespace transm {

namespace {

/* Feeds months 1..last with the given seconds per month. */
RunTimePredictor Predict(int total_months, int last_month, double (*seconds)(int))
{
    RunTimePredictor predictor;
    predictor.SetTotalMonths(TimeSpan(0, total_months));
    for (int month = 1; month <= last_month; month++)
        predictor.Update(std::make_pair(month, seconds(month)));
    return predictor;
}

void ExpectWellFormed(const std::string &bar, std::size_t length)
{
    ASSERT_EQ(bar.size(), length + 3);
    EXPECT_EQ(bar.front(), '[');
    EXPECT_EQ(bar.back(), ']');
    EXPECT_EQ(bar.find('|'), bar.rfind('|'));
}

} // namespace

TEST(RunTimePredictorTest, ProgressBarSurvivesTheClockSteppingBack)
{
    /* Month times come from the wall clock, which can step back (WSL resyncs it).
     * One negative month time makes every later estimate NaN; the bar used to turn
     * that into a length near 2^64 and abort the run with std::length_error. */
    auto predictor = Predict(840, 815, [](int month) { return month == 811 ? -0.5 : 0.08; });
    ASSERT_TRUE(std::isnan(predictor.GetEstimatedPercentComplete()));
    std::string bar;
    ASSERT_NO_THROW(bar = predictor.MakeProgressBar(40));
    ExpectWellFormed(bar, 40);
}

TEST(RunTimePredictorTest, ProgressBarIsFullWhenEstimatePassesTheEnd)
{
    /* Running past the planned months puts the estimate above 1. */
    auto predictor = Predict(100, 200, [](int) { return 1.0; });
    ASSERT_GT(predictor.GetEstimatedPercentComplete(), 1.0);
    std::string bar;
    ASSERT_NO_THROW(bar = predictor.MakeProgressBar(40));
    EXPECT_EQ(bar, "[" + std::string(40, '=') + "|]");
}

TEST(RunTimePredictorTest, ProgressBarIsEmptyBeforeAnyEstimate)
{
    RunTimePredictor predictor;
    predictor.SetTotalMonths(TimeSpan(0, 840));
    EXPECT_EQ(predictor.MakeProgressBar(40), "[|" + std::string(40, '=') + "]");
}

TEST(RunTimePredictorTest, ProgressBarSurvivesZeroTimings)
{
    /* log(0) in the fit gives a NaN estimate. */
    auto predictor = Predict(840, 100, [](int) { return 0.0; });
    std::string bar;
    ASSERT_NO_THROW(bar = predictor.MakeProgressBar(40));
    ExpectWellFormed(bar, 40);
}

TEST(RunTimePredictorTest, ProgressBarFollowsAnInRangeEstimate)
{
    auto predictor = Predict(840, 420, [](int) { return 1.0; });
    double percent = predictor.GetEstimatedPercentComplete();
    ASSERT_GT(percent, 0.0);
    ASSERT_LT(percent, 1.0);
    std::string bar = predictor.MakeProgressBar(40);
    ExpectWellFormed(bar, 40);
    EXPECT_EQ(bar.find('|') - 1, (std::size_t)(percent * 40));
}

} // namespace transm
