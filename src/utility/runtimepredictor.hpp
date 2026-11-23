#ifndef RUNTIMEPREDICTOR_HPP
#define RUNTIMEPREDICTOR_HPP

#include <cmath>
#include <cstddef>
#include <string>

#include "utility/time.hpp"

namespace transm {

/**
 * Estimates how long a run has left from how long its months have taken so far.
 *
 * Progress is the share of months finished, which is exact. The time left is an
 * estimate: the months left, times the recent seconds per month. "Recent" is an
 * average in which a month's weight halves every HalfLifeMonths, so the estimate
 * follows the run when months get slower (a growing population, an intervention
 * starting, another job on the machine) and is not thrown by one slow month.
 *
 * Against the month timings of six Miami runs (2,000 to 100,000 people), half-lives
 * from 12 to 200 months and fitted trends all came within two points of each other,
 * so the simplest is used. What is left of the error comes from months ahead being
 * slower or faster than any seen so far, which no estimate from the past can know.
 */
class RunTimePredictor
{
public:
    /** A month's weight in the average halves after this many months. */
    static constexpr double HalfLifeMonths = 36.0;

    /** Months to time before estimating how long is left. */
    static constexpr int MonthsBeforeEstimate = 12;

    /** Once there is an estimate, one month counts in the average as at most this many
     * times the recent pace, so a stall (the machine swapping, another job) does not
     * inflate the estimate for years. Longer slowdowns still raise it month by month. */
    static constexpr double MostTimesRecentPace = 4.0;

    RunTimePredictor() = default;

    RunTimePredictor &operator=(const RunTimePredictor &) = delete;

    void SetTotalMonths(TimeSpan months)
    {
        total_months_ = months.in_months();
    }

    /**
     * Record a finished month.
     *
     * @param months_done months finished so far, this one included
     * @param seconds how long this month took. Not a number, infinite or negative (a
     *                clock that stepped back) counts as no time. */
    void Update(int months_done, double seconds)
    {
        if (!(seconds >= 0) || std::isinf(seconds))
            seconds = 0;

        months_done_ = months_done;
        elapsed_seconds_ += seconds;
        observations_++;

        double counted = seconds;
        if (observations_ > MonthsBeforeEstimate && weighted_seconds_ > 0) {
            double most = MostTimesRecentPace * GetSecondsPerMonth();
            if (counted > most)
                counted = most;
        }

        const double keep = std::pow(0.5, 1.0 / HalfLifeMonths);
        weight_ = keep * weight_ + 1;
        weighted_seconds_ = keep * weighted_seconds_ + counted;
    }

    int GetMonthsDone() const { return months_done_; }

    int GetTotalMonths() const { return total_months_; }

    /** Share of the run's months that are finished, from 0 to 1. */
    double GetFractionComplete() const
    {
        if (total_months_ <= 0)
            return 0;
        double fraction = (double)months_done_ / total_months_;
        return fraction > 0 ? (fraction < 1 ? fraction : 1) : 0;
    }

    double GetElapsedSeconds() const { return elapsed_seconds_; }

    /** False until enough months have been timed to say how long is left. */
    bool HasEstimate() const
    {
        return observations_ >= MonthsBeforeEstimate && weighted_seconds_ > 0;
    }

    /** Recent seconds per month; 0 before any month is timed. */
    double GetSecondsPerMonth() const
    {
        return weight_ > 0 ? weighted_seconds_ / weight_ : 0;
    }

    /** Recent speed in simulated months per second; 0 when unknown. */
    double GetMonthsPerSecond() const
    {
        double seconds = GetSecondsPerMonth();
        return seconds > 0 ? 1.0 / seconds : 0;
    }

    /** Estimated seconds until the last month is finished; 0 without an estimate. */
    double GetEstimatedTimeRemaining() const
    {
        int months_left = total_months_ - months_done_;
        if (!HasEstimate() || months_left <= 0)
            return 0;
        return months_left * GetSecondsPerMonth();
    }

    double GetEstimatedRunTime() const
    {
        return elapsed_seconds_ + GetEstimatedTimeRemaining();
    }

    /** "[#####...............]", filled by the share of months finished. */
    std::string MakeProgressBar(std::size_t bar_length) const
    {
        std::size_t filled = (std::size_t)(GetFractionComplete() * bar_length);
        if (filled > bar_length)
            filled = bar_length;
        return "[" + std::string(filled, '#') + std::string(bar_length - filled, '.') + "]";
    }

private:
    int total_months_ = 0;
    int months_done_ = 0;
    int observations_ = 0;
    double elapsed_seconds_ = 0;
    double weight_ = 0;                 // sum of the months' weights
    double weighted_seconds_ = 0;       // sum of weight times seconds
};

} // namespace transm


#endif /* RUNTIMEPREDICTOR_HPP */
