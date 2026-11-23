#ifndef PROGRESSREPORT_HPP
#define PROGRESSREPORT_HPP

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace transm {

/** What a progress report says about a run at one month. */
struct ProgressSnapshot
{
    int months_done = 0;
    int total_months = 0;
    double elapsed_seconds = 0;
    bool has_estimate = false;          // false while too few months have been timed
    double seconds_remaining = 0;
    double months_per_second = 0;
    std::size_t people_alive = 0;
    double hiv_prevalence = -1;         // share of sexually active people, 0 to 1; negative leaves it out
};

/** Elapsed time as it reads on a clock: "0:42", "12:05", "3:02:10". */
inline std::string FormatDuration(double seconds)
{
    if (!(seconds >= 0) || std::isinf(seconds))
        seconds = 0;
    long total = (long)(seconds + 0.5);
    char text[32];
    if (total >= 3600)
        std::snprintf(text, sizeof text, "%ld:%02ld:%02ld", total / 3600, total / 60 % 60, total % 60);
    else
        std::snprintf(text, sizeof text, "%ld:%02ld", total / 60, total % 60);
    return text;
}

/**
 * Time left as a rounded phrase: "about 40 s", "about 25 min", "about 2 h 10 min".
 * It is an estimate, so it is not given to the second. */
inline std::string FormatTimeLeft(double seconds)
{
    if (!(seconds >= 0) || std::isinf(seconds))
        seconds = 0;
    char text[48];
    if (seconds < 10)
        return "a few seconds";
    if (seconds < 90)
        std::snprintf(text, sizeof text, "about %ld s", (long)(seconds / 5 + 0.5) * 5);
    else if (seconds < 90 * 60)
        std::snprintf(text, sizeof text, "about %ld min", (long)(seconds / 60 + 0.5));
    else {
        long minutes = (long)(seconds / 300 + 0.5) * 5;
        if (minutes % 60 == 0)
            std::snprintf(text, sizeof text, "about %ld h", minutes / 60);
        else
            std::snprintf(text, sizeof text, "about %ld h %ld min", minutes / 60, minutes % 60);
    }
    return text;
}

/** 1234567 as "1,234,567". */
inline std::string FormatCount(std::size_t count)
{
    std::string digits = std::to_string(count);
    for (int at = (int)digits.size() - 3; at > 0; at -= 3)
        digits.insert((std::size_t)at, ",");
    return digits;
}

/** Speed with enough digits to see it change: "3.4 months/s", "12 months/s", "0.08 months/s". */
inline std::string FormatSpeed(double months_per_second)
{
    char text[40];
    if (months_per_second >= 10)
        std::snprintf(text, sizeof text, "%.0f months/s", months_per_second);
    else if (months_per_second >= 1)
        std::snprintf(text, sizeof text, "%.1f months/s", months_per_second);
    else
        std::snprintf(text, sizeof text, "%.2f months/s", months_per_second);
    return text;
}

/**
 * The parts of a progress report after the month count, most useful first, so a narrow
 * terminal can drop the last ones. */
inline std::vector<std::string> ProgressDetails(const ProgressSnapshot &now)
{
    std::vector<std::string> parts;
    if (now.months_done >= now.total_months)
        parts.push_back("done");
    else if (now.has_estimate)
        parts.push_back(FormatTimeLeft(now.seconds_remaining) + " left");
    else
        parts.push_back("estimating time left");
    parts.push_back(FormatDuration(now.elapsed_seconds) + " elapsed");
    if (now.months_per_second > 0)
        parts.push_back(FormatSpeed(now.months_per_second));
    if (now.people_alive > 0)
        parts.push_back(FormatCount(now.people_alive) + " alive");
    if (now.hiv_prevalence >= 0) {
        char text[40];
        std::snprintf(text, sizeof text, "HIV prevalence %.2f%%", 100 * now.hiv_prevalence);
        parts.push_back(text);
    }
    return parts;
}

inline std::string MonthOfTotal(const ProgressSnapshot &now)
{
    return "month " + std::to_string(now.months_done) + "/" + std::to_string(now.total_months);
}

inline int PercentComplete(const ProgressSnapshot &now)
{
    if (now.total_months <= 0)
        return 0;
    int percent = (int)(100.0 * now.months_done / now.total_months);       // 100 only when finished
    return percent < 0 ? 0 : (percent > 100 ? 100 : percent);
}

/**
 * One line for a log file: no bar and no control characters, the same fields in the
 * same order every time so it can be searched and compared.
 *
 *   month  504/1200   42%   about 3 min left   2:31 elapsed   3.4 months/s   10,579 alive   HIV prevalence 0.62% */
inline std::string FormatProgressLogLine(const ProgressSnapshot &now)
{
    std::string total = std::to_string(now.total_months);
    std::string done = std::to_string(now.months_done);
    if (done.size() < total.size())
        done.insert(0, total.size() - done.size(), ' ');
    char percent[16];
    std::snprintf(percent, sizeof percent, "%3d%%", PercentComplete(now));
    std::string line = "month " + done + "/" + total + "  " + percent;
    for (const auto &part : ProgressDetails(now))
        line += "   " + part;
    return line;
}

/**
 * One line for a terminal, redrawn in place: a bar, the percentage, the month, then as
 * many details as fit in `columns`. A line longer than the terminal would wrap and
 * could no longer be redrawn.
 *
 *   [########............]  42%  month 504/1200  about 3 min left  2:31 elapsed  3.4 months/s */
inline std::string FormatProgressBarLine(const ProgressSnapshot &now, std::size_t columns)
{
    const std::size_t bar_length = columns >= 70 ? 20 : 10;
    double fraction = now.total_months > 0 ? (double)now.months_done / now.total_months : 0;
    fraction = fraction > 0 ? (fraction < 1 ? fraction : 1) : 0;
    std::size_t filled = (std::size_t)(fraction * bar_length);
    char percent[16];
    std::snprintf(percent, sizeof percent, "%3d%%", PercentComplete(now));
    std::string line = "[" + std::string(filled, '#') + std::string(bar_length - filled, '.') + "] " + percent +
                       "  " + MonthOfTotal(now);
    for (const auto &part : ProgressDetails(now)) {
        if (line.size() + 2 + part.size() > columns - 1)       // leave the last column free: some terminals wrap on it
            break;
        line += "  " + part;
    }
    return line;
}

inline bool StandardOutputIsATerminal()
{
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

/** Width of the terminal on standard output, or 80 when it cannot be asked. */
inline std::size_t TerminalColumns()
{
#ifndef _WIN32
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col >= 20)
        return size.ws_col;
#endif
    return 80;
}

/**
 * Decides when to report progress and in which form.
 *
 *   Bar    on a terminal: one line, redrawn in place a few times a second.
 *   Lines  in a log file or pipe: a plain line every simulated year, and at least once
 *          a minute, so the log stays short and shows where a run stopped.
 *   Off    nothing.
 */
class ProgressReporter
{
public:
    enum class Mode { Off, Lines, Bar };

    static constexpr double SecondsBetweenRedraws = 0.2;
    static constexpr int MonthsBetweenLines = 12;
    static constexpr double MostSecondsBetweenLines = 60;

    /**
     * @param on_terminal standard output is a terminal
     * @param other_output the run prints other lines every month (FOCUS does), which a
     *                     redrawn bar would be mixed into
     * @param requested the TRANSM_PROGRESS setting: "off", "lines", "bar" or anything
     *                  else (also null) to choose by the first two */
    static Mode ChooseMode(bool on_terminal, bool other_output, const char *requested)
    {
        std::string choice = requested ? requested : "";
        if (choice == "off" || choice == "none" || choice == "0")
            return Mode::Off;
        if (choice == "lines")
            return Mode::Lines;
        if (choice == "bar")
            return Mode::Bar;
        return on_terminal && !other_output ? Mode::Bar : Mode::Lines;
    }

    explicit ProgressReporter(std::ostream &out = std::cout) : out_(&out) {}

    void SetMode(Mode mode) { mode_ = mode; }

    Mode GetMode() const { return mode_; }

    /** For tests: the width used for the bar instead of asking the terminal. */
    void SetColumns(std::size_t columns) { columns_ = columns; }

    /**
     * Report after a finished month, if it is time to.
     *
     * @param clock_seconds a clock that only moves forward, in seconds */
    void Report(const ProgressSnapshot &now, double clock_seconds)
    {
        if (mode_ == Mode::Off)
            return;

        bool last = now.months_done >= now.total_months;
        double waited = clock_seconds - last_report_seconds_;

        if (mode_ == Mode::Bar) {
            if (reported_ && !last && waited < SecondsBetweenRedraws)
                return;
            std::string line = FormatProgressBarLine(now, columns_ ? columns_ : TerminalColumns());
            /* Carriage return, the line, then clear what an earlier, longer line left. */
            *out_ << "\r" << line << "\033[K" << std::flush;
            bar_drawn_ = true;
        } else {
            bool year_done = now.months_done % MonthsBetweenLines == 0;
            if (!last && !year_done && !(reported_ && waited >= MostSecondsBetweenLines))
                return;
            *out_ << FormatProgressLogLine(now) << std::endl;
        }

        reported_ = true;
        last_report_seconds_ = clock_seconds;
    }

    /**
     * Close the report when the run stops: end the redrawn line, and say what was done.
     *
     * @param completed false when the run stopped before its last month */
    void Finish(const ProgressSnapshot &now, bool completed)
    {
        if (mode_ == Mode::Off)
            return;
        if (bar_drawn_)
            *out_ << "\n";
        std::string speed = now.elapsed_seconds > 0 && now.months_done > 0
                                ? " (" + FormatSpeed(now.months_done / now.elapsed_seconds) + ")"
                                : "";
        if (completed)
            *out_ << "Simulated " << now.months_done << " months in " << FormatDuration(now.elapsed_seconds) << speed
                  << "; writing final outputs." << std::endl;
        else
            *out_ << "Stopped at month " << now.months_done << " of " << now.total_months << " after "
                  << FormatDuration(now.elapsed_seconds) << "." << std::endl;
        bar_drawn_ = false;
    }

private:
    std::ostream *out_;
    Mode mode_ = Mode::Lines;
    std::size_t columns_ = 0;
    bool reported_ = false;
    bool bar_drawn_ = false;
    double last_report_seconds_ = 0;
};

} // namespace transm

#endif /* PROGRESSREPORT_HPP */
