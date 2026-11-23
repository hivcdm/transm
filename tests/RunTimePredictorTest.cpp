#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <sstream>
#include <string>

#include "utility/time.hpp"
#include "utility/runtimepredictor.hpp"
#include "utility/progressreport.hpp"

namespace transm {

namespace {

/* Feeds months 1..last with the given seconds per month. */
RunTimePredictor Predict(int total_months, int last_month, double (*seconds)(int))
{
    RunTimePredictor predictor;
    predictor.SetTotalMonths(TimeSpan(0, total_months));
    for (int month = 1; month <= last_month; month++)
        predictor.Update(month, seconds(month));
    return predictor;
}

/* What months first+1..last really take, to compare an estimate with. */
double TrueSeconds(int first, int last, double (*seconds)(int))
{
    double sum = 0;
    for (int month = first + 1; month <= last; month++)
        sum += seconds(month);
    return sum;
}

ProgressSnapshot Snapshot(int month, int total = 1200)
{
    ProgressSnapshot now;
    now.months_done = month;
    now.total_months = total;
    now.elapsed_seconds = 151;
    now.has_estimate = true;
    now.seconds_remaining = 200;
    now.months_per_second = 3.4;
    now.people_alive = 10579;
    now.hiv_prevalence = 0.0062;
    return now;
}

} // namespace

/* ---- how long is left ---- */

TEST(RunTimePredictorTest, ProgressIsTheShareOfMonthsFinished)
{
    auto predictor = Predict(840, 210, [](int) { return 1.0; });
    EXPECT_DOUBLE_EQ(predictor.GetFractionComplete(), 0.25);
    EXPECT_EQ(predictor.GetMonthsDone(), 210);
    EXPECT_DOUBLE_EQ(predictor.GetElapsedSeconds(), 210.0);
}

TEST(RunTimePredictorTest, SteadyMonthsGiveTheExactTimeLeft)
{
    auto predictor = Predict(840, 420, [](int) { return 0.25; });
    EXPECT_NEAR(predictor.GetEstimatedTimeRemaining(), 420 * 0.25, 1e-6);
    EXPECT_NEAR(predictor.GetMonthsPerSecond(), 4.0, 1e-6);
    EXPECT_NEAR(predictor.GetEstimatedRunTime(), 840 * 0.25, 1e-6);
}

TEST(RunTimePredictorTest, NoEstimateUntilEnoughMonthsAreTimed)
{
    auto early = Predict(840, RunTimePredictor::MonthsBeforeEstimate - 1, [](int) { return 1.0; });
    EXPECT_FALSE(early.HasEstimate());
    EXPECT_EQ(early.GetEstimatedTimeRemaining(), 0);
    auto later = Predict(840, RunTimePredictor::MonthsBeforeEstimate, [](int) { return 1.0; });
    EXPECT_TRUE(later.HasEstimate());
}

TEST(RunTimePredictorTest, RecentMonthsCountMoreThanEarlyOnes)
{
    /* Each month a little slower than the last, as when the population grows. The
     * average over the whole run so far promises too early a finish; the recent
     * average comes closer. */
    auto slowing = [](int month) { return 0.1 + 0.0005 * month; };
    auto predictor = Predict(1200, 400, slowing);
    double truth = TrueSeconds(400, 1200, slowing);
    double at_average_speed = 800 * predictor.GetElapsedSeconds() / 400;
    double estimate = predictor.GetEstimatedTimeRemaining();
    EXPECT_GT(estimate, at_average_speed);
    EXPECT_LT(std::abs(estimate - truth), std::abs(at_average_speed - truth));
}

TEST(RunTimePredictorTest, AStepInSpeedIsFollowedWithinAFewYears)
{
    /* Months triple in cost from month 300 on, as when an intervention starts. */
    auto step = [](int month) { return month < 300 ? 0.1 : 0.3; };
    auto predictor = Predict(1200, 480, step);
    double truth = TrueSeconds(480, 1200, step);
    EXPECT_NEAR(predictor.GetEstimatedTimeRemaining(), truth, 0.05 * truth);
}

TEST(RunTimePredictorTest, OneSlowMonthDoesNotThrowTheEstimate)
{
    auto hiccup = [](int month) { return month == 400 ? 30.0 : 0.25; };
    auto predictor = Predict(1200, 400, hiccup);
    EXPECT_NEAR(predictor.GetEstimatedTimeRemaining(), 800 * 0.25, 0.6 * 800 * 0.25);
    auto later = Predict(1200, 500, hiccup);
    EXPECT_NEAR(later.GetEstimatedTimeRemaining(), 700 * 0.25, 0.1 * 700 * 0.25);
}

TEST(RunTimePredictorTest, MonthsThatGetFasterStillLeaveTimeToRun)
{
    auto speeding_up = [](int month) { return month < 200 ? 2.0 - 0.0099 * month : 0.02; };
    for (int month : {50, 150, 199, 205, 400}) {
        auto predictor = Predict(1200, month, speeding_up);
        EXPECT_GT(predictor.GetEstimatedTimeRemaining(), 0) << "month " << month;
    }
}

TEST(RunTimePredictorTest, BadTimingsCountAsNoTime)
{
    /* The timer no longer steps back, but a timing that is negative, infinite or not a
     * number must still leave every figure finite. One such month used to make every
     * later estimate NaN and abort the run when the bar was drawn. */
    auto predictor = Predict(840, 815, [](int month) {
        return month == 811 ? -0.5 : month == 812 ? std::numeric_limits<double>::quiet_NaN()
             : month == 813 ? std::numeric_limits<double>::infinity() : 0.08;
    });
    EXPECT_TRUE(std::isfinite(predictor.GetEstimatedTimeRemaining()));
    EXPECT_TRUE(std::isfinite(predictor.GetElapsedSeconds()));
    EXPECT_NEAR(predictor.GetElapsedSeconds(), 812 * 0.08, 1e-9);
    EXPECT_GE(predictor.GetEstimatedTimeRemaining(), 0);
}

TEST(RunTimePredictorTest, InstantMonthsGiveNoEstimateAndNoFault)
{
    auto predictor = Predict(840, 100, [](int) { return 0.0; });
    EXPECT_FALSE(predictor.HasEstimate());
    EXPECT_EQ(predictor.GetEstimatedTimeRemaining(), 0);
    EXPECT_EQ(predictor.GetMonthsPerSecond(), 0);
}

TEST(RunTimePredictorTest, NothingLeftAtOrPastTheLastMonth)
{
    auto finished = Predict(100, 100, [](int) { return 1.0; });
    EXPECT_EQ(finished.GetEstimatedTimeRemaining(), 0);
    EXPECT_DOUBLE_EQ(finished.GetFractionComplete(), 1.0);
    auto past = Predict(100, 200, [](int) { return 1.0; });
    EXPECT_EQ(past.GetEstimatedTimeRemaining(), 0);
    EXPECT_DOUBLE_EQ(past.GetFractionComplete(), 1.0);
}

TEST(RunTimePredictorTest, ProgressBarIsFilledByMonthsFinished)
{
    RunTimePredictor fresh;
    fresh.SetTotalMonths(TimeSpan(0, 840));
    EXPECT_EQ(fresh.MakeProgressBar(40), "[" + std::string(40, '.') + "]");
    EXPECT_EQ(Predict(840, 420, [](int) { return 1.0; }).MakeProgressBar(40),
              "[" + std::string(20, '#') + std::string(20, '.') + "]");
    EXPECT_EQ(Predict(100, 200, [](int) { return 1.0; }).MakeProgressBar(40), "[" + std::string(40, '#') + "]");
    RunTimePredictor no_total;
    EXPECT_EQ(no_total.MakeProgressBar(10), "[..........]");
}

/* ---- how it is written ---- */

TEST(ProgressReportTest, DurationsReadLikeAClock)
{
    EXPECT_EQ(FormatDuration(0), "0:00");
    EXPECT_EQ(FormatDuration(42), "0:42");
    EXPECT_EQ(FormatDuration(151), "2:31");
    EXPECT_EQ(FormatDuration(3599.6), "1:00:00");
    EXPECT_EQ(FormatDuration(10930), "3:02:10");
    EXPECT_EQ(FormatDuration(-5), "0:00");
    EXPECT_EQ(FormatDuration(std::numeric_limits<double>::quiet_NaN()), "0:00");
}

TEST(ProgressReportTest, TimeLeftIsRoundedBecauseItIsAnEstimate)
{
    EXPECT_EQ(FormatTimeLeft(4), "a few seconds");
    EXPECT_EQ(FormatTimeLeft(42), "about 40 s");
    EXPECT_EQ(FormatTimeLeft(200), "about 3 min");
    EXPECT_EQ(FormatTimeLeft(25 * 60 + 10), "about 25 min");
    EXPECT_EQ(FormatTimeLeft(2 * 3600 + 11 * 60), "about 2 h 10 min");
    EXPECT_EQ(FormatTimeLeft(3 * 3600 + 60), "about 3 h");
    EXPECT_EQ(FormatTimeLeft(std::numeric_limits<double>::quiet_NaN()), "a few seconds");
}

TEST(ProgressReportTest, CountsAndSpeeds)
{
    EXPECT_EQ(FormatCount(0), "0");
    EXPECT_EQ(FormatCount(999), "999");
    EXPECT_EQ(FormatCount(10579), "10,579");
    EXPECT_EQ(FormatCount(1234567), "1,234,567");
    EXPECT_EQ(FormatSpeed(12.3), "12 months/s");
    EXPECT_EQ(FormatSpeed(3.44), "3.4 months/s");
    EXPECT_EQ(FormatSpeed(0.083), "0.08 months/s");
}

TEST(ProgressReportTest, LogLineHasEveryFieldAndNoControlCharacters)
{
    std::string line = FormatProgressLogLine(Snapshot(504));
    EXPECT_EQ(line, "month  504/1200   42%   about 3 min left   2:31 elapsed   3.4 months/s   10,579 alive   "
                    "HIV prevalence 0.62%");
    for (char c : line)
        EXPECT_GE(c, ' ');
}

TEST(ProgressReportTest, PercentReaches100OnlyAtTheLastMonth)
{
    EXPECT_NE(FormatProgressLogLine(Snapshot(1199)).find(" 99%"), std::string::npos);
    std::string last = FormatProgressLogLine(Snapshot(1200));
    EXPECT_NE(last.find("100%"), std::string::npos);
    EXPECT_NE(last.find("done"), std::string::npos);
    EXPECT_EQ(last.find("left"), std::string::npos);
}

TEST(ProgressReportTest, SaysSoWhileThereIsNoEstimate)
{
    auto now = Snapshot(5);
    now.has_estimate = false;
    now.hiv_prevalence = -1;        // no sexually active people yet
    std::string line = FormatProgressLogLine(now);
    EXPECT_NE(line.find("estimating time left"), std::string::npos);
    EXPECT_EQ(line.find("prevalence"), std::string::npos);
}

TEST(ProgressReportTest, BarLineNeverExceedsTheTerminal)
{
    for (std::size_t columns : {40u, 60u, 80u, 100u, 200u}) {
        std::string line = FormatProgressBarLine(Snapshot(504), columns);
        EXPECT_LT(line.size(), columns) << line;
        EXPECT_NE(line.find("month 504/1200"), std::string::npos) << line;
    }
    EXPECT_EQ(FormatProgressBarLine(Snapshot(504), 200),
              "[########............]  42%  month 504/1200  about 3 min left  2:31 elapsed  3.4 months/s  "
              "10,579 alive  HIV prevalence 0.62%");
    /* A narrow terminal keeps the time left and drops the rest. */
    std::string narrow = FormatProgressBarLine(Snapshot(504), 60);
    EXPECT_NE(narrow.find("about 3 min left"), std::string::npos);
    EXPECT_EQ(narrow.find("alive"), std::string::npos);
}

TEST(ProgressReportTest, ModeFollowsTheTerminalUnlessTold)
{
    using Mode = ProgressReporter::Mode;
    EXPECT_EQ(ProgressReporter::ChooseMode(true, false, nullptr), Mode::Bar);
    EXPECT_EQ(ProgressReporter::ChooseMode(false, false, nullptr), Mode::Lines);
    EXPECT_EQ(ProgressReporter::ChooseMode(true, true, nullptr), Mode::Lines);     // FOCUS prints every month
    EXPECT_EQ(ProgressReporter::ChooseMode(true, false, "lines"), Mode::Lines);
    EXPECT_EQ(ProgressReporter::ChooseMode(false, false, "bar"), Mode::Bar);
    EXPECT_EQ(ProgressReporter::ChooseMode(true, false, "off"), Mode::Off);
    EXPECT_EQ(ProgressReporter::ChooseMode(false, false, "anything else"), Mode::Lines);
}

TEST(ProgressReportTest, LogGetsALineAYearAndTheLastMonth)
{
    std::ostringstream out;
    ProgressReporter reporter(out);
    reporter.SetMode(ProgressReporter::Mode::Lines);
    for (int month = 1; month <= 30; month++)
        reporter.Report(Snapshot(month, 30), 0.1 * month);
    reporter.Finish(Snapshot(30, 30), true);
    std::string text = out.str();
    int lines = 0;
    for (char c : text)
        lines += c == '\n';
    EXPECT_EQ(lines, 4);                                           // months 12, 24, 30 and the summary
    EXPECT_NE(text.find("month 12/30"), std::string::npos);
    EXPECT_NE(text.find("month 24/30"), std::string::npos);
    EXPECT_NE(text.find("month 30/30"), std::string::npos);
    EXPECT_NE(text.find("Simulated 30 months in 2:31"), std::string::npos);
    EXPECT_EQ(text.find('\r'), std::string::npos);
    EXPECT_EQ(text.find('\033'), std::string::npos);
}

TEST(ProgressReportTest, SlowRunsStillReportAtLeastOnceAMinute)
{
    std::ostringstream out;
    ProgressReporter reporter(out);
    reporter.SetMode(ProgressReporter::Mode::Lines);
    for (int month = 12; month <= 23; month++)                     // 25 s a month
        reporter.Report(Snapshot(month), 25.0 * month);
    std::string text = out.str();
    EXPECT_NE(text.find("month   12/1200"), std::string::npos);
    EXPECT_NE(text.find("month   15/1200"), std::string::npos);    // 75 s after month 12
    EXPECT_EQ(text.find("month   13/1200"), std::string::npos);
}

TEST(ProgressReportTest, BarIsRedrawnInPlaceAndNotTooOften)
{
    std::ostringstream out;
    ProgressReporter reporter(out);
    reporter.SetMode(ProgressReporter::Mode::Bar);
    reporter.SetColumns(100);
    for (int month = 1; month <= 100; month++)                     // 0.01 s a month: 1 s in all
        reporter.Report(Snapshot(month, 100), 0.01 * month);
    reporter.Finish(Snapshot(100, 100), true);
    std::string text = out.str();
    int redraws = 0, newlines = 0;
    for (char c : text) {
        redraws += c == '\r';
        newlines += c == '\n';
    }
    EXPECT_GE(redraws, 4);
    EXPECT_LE(redraws, 7);                                         // about five a second, plus the first and last
    EXPECT_EQ(newlines, 2);                                        // end of the bar, then the summary
    EXPECT_NE(text.find("100%  month 100/100  done"), std::string::npos);
}

TEST(ProgressReportTest, OffPrintsNothingAndAStoppedRunSaysWhere)
{
    std::ostringstream quiet;
    ProgressReporter off(quiet);
    off.SetMode(ProgressReporter::Mode::Off);
    off.Report(Snapshot(12), 1);
    off.Finish(Snapshot(12), true);
    EXPECT_EQ(quiet.str(), "");

    std::ostringstream out;
    ProgressReporter reporter(out);
    reporter.Finish(Snapshot(648), false);
    EXPECT_EQ(out.str(), "Stopped at month 648 of 1200 after 2:31.\n");
}

} // namespace transm
